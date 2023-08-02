describe 'database' do
    before do
        `rm -rf test.db`
    end

    def run_script(commands)
        raw_output = nil
        IO.popen("./a.out test.db", "r+") do |pipe|
            commands.each do |command|
                begin
                    pipe.puts command
                rescue Errno::EPIPE
                    break
                end
            end

            pipe.close_write

            raw_output = pipe.gets(nil)
        end

        raw_output.split("\n")
    end

    it 'insère et trouve une ligne' do
        result = run_script([
            "insère 1 user1 person1@example.com",
            "sélectionne",
            ".sors"
        ])

        expect(result).to match_array([
            "db > Exécution réussie.",
            "db > (1, user1, person1@example.com)",
            "Exécution réussie.",
            "db > ",
        ])
    end

    it "imprime une erreur quand la table est pleine" do
        script = (1 .. 1401).map do |i|
            "insère #{i} user#{i} person#{i}@example.com"
        end
        script << ".sors"

        result = run_script(script)

        expect(result[-2]).to eq("db > Erreur: table pleine.")
    end

    it "permet d'insérer des chaines dont la longueur est maximale" do
        nom_long = "a" * 32
        adresse_longue = "a" * 255

        script = [
            "insère 1 #{nom_long} #{adresse_longue}",
            "sélectionne",
            ".sors"
        ]

        result = run_script(script)

        expect(result).to match_array([
            "db > Exécution réussie.",
            "db > (1, #{nom_long}, #{adresse_longue})",
            "Exécution réussie.",
            "db > ",
        ])
    end

    it "imprime une erreur si une chaine est trop longue" do
        nom_long = "a" * 33
        adresse_longue = "a" * 256

        script = [
            "insère 1 #{nom_long} #{adresse_longue}",
            "sélectionne",
            ".sors"
        ]

        result = run_script(script)

        expect(result).to match_array([
            "db > La chaine est trop longue.",
            "db > Exécution réussie.",
            "db > ",
        ])
    end

    it "imprime une erreur si l'id est négatif" do
        script = [
            "insère -1 pierre exemple@courriel.fr",
            "sélectionne",
            ".sors"
        ]

        result = run_script(script)

        expect(result).to match_array([
            "db > L'ID doit être positif.",
            "db > Exécution réussie.",
            "db > ",
        ])
    end

    # À FAIRE : test pour voir si l'ID est bel et bien un nombre entier

    it "les données persistent après de la cloture du programme" do
        result1 = run_script([
            "insère 1 pierre exemple@courriel.fr",
            ".sors"
        ])

        expect(result1).to match_array([
            "db > Exécution réussie.",
            "db > "
        ])

        result1 = run_script([
            "sélectionne",
            ".sors"
        ])

        expect(result1).to match_array([
            "db > (1, pierre, exemple@courriel.fr)",
            "Exécution réussie.",
            "db > "
        ])
    end
end